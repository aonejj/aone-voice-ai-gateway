//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCClientImpl"

#include <cassert>

#include <utils/Log.h>

#include "rtc_base/logging.h"
#include "api/media_types.h"
#include "api/rtp_parameters.h"
#include "api/jsep_session_description.h"
#include "rtc_base/helpers.h"
#include "media/base/stream_params.h"
#include "api/jsep.h" 
#include "pc/session_description.h"
#include "media/base/codec.h" 

#include "RTCClientAudioRtpReceiver.h"
#include "RTCClientAudioSender.h"
#include "RTCAudioSenderSchedulerInterface.h"
#include "RTCClientImpl.h"


#include "../RTCPeerConnection/rtc_pc_src/RTCFunctionTracer.h"

#define _ENABLE_SDP_LOG__

std::shared_ptr<RTCClientInterface> RTCClientImpl::Create(const std::string &id, 
									 std::vector<RTCIceServerInfo>& rtcIceInfos,
									 RTCClientListener* listener,
									 RTCAudioSenderSchedulerInterface* audio_sender_scheduler) {
    return std::shared_ptr<RTCClientImpl>(
        new RTCClientImpl(
            id,
            rtcIceInfos,
            listener,
            audio_sender_scheduler
        )
    );
}

bool RTCClientImpl::InitializeSSL() {
	return webrtc::RTCPeer::InitializeSSL();
}

bool RTCClientImpl::CleanupSSL() {
	return webrtc::RTCPeer::CleanupSSL();
}

RTCClientImpl::RTCClientImpl(const std::string &id, std::vector<RTCIceServerInfo>& rtcIceInfos,
			     RTCClientListener* listener, 
				 RTCAudioSenderSchedulerInterface* audio_sender_scheduler) 
 : _id(id), 
   _listener(listener),
   _rtc_peer(nullptr), 
   _local_desc(nullptr),
   _audio_sender_scheduler(audio_sender_scheduler) {
	_state = RTCClientInterface::RTC_CLIENT_STATE_UNINIT;

//	RTC_CLASS_FUNCTION_TRACER("RTCClient::RTCClient", this);
	if (!rtcIceInfos.empty()) {
		for (auto it : rtcIceInfos) {
			webrtc::PeerConnectionInterfaceDefs::IceServer ice_info;
			ice_info.urls.push_back(it._uri);
			if (!it._is_stun) {
				ice_info.username = it._user_name;
				ice_info.password = it._credential;
			}
			_ice_servers.push_back(ice_info);
		}
	}

	_state = RTCClientInterface::RTC_CLIENT_STATE_INIT;
}

RTCClientImpl::~RTCClientImpl() {
//	RTC_CLASS_FUNCTION_TRACER("RTCClient::~RTCClient", this);
}

// RTCPeerInterface methode
webrtc::RTCPeer* RTCClientImpl::PeerPtr() {
	return _rtc_peer.get();
}

status_t RTCClientImpl::CreatePeerConnection() {
	RTC_CLASS_FUNCTION_TRACER("RTCPeerClient::CreatePeerConnection", this);
	_rtc_peer = webrtc::RTCPeer::Create(this);
	if (_rtc_peer == nullptr) {
		_state = RTCClientInterface::RTC_CLIENT_STATE_ERROR;
		return INVALID_OPERATION;
	}

	webrtc::PeerConnectionInterfaceDefs::RTCConfiguration rtcConfigure;
	rtcConfigure.type = webrtc::PeerConnectionInterfaceDefs::kAll;
	rtcConfigure.offer_extmap_allow_mixed = true;
	rtcConfigure.bundle_policy = webrtc::PeerConnectionInterfaceDefs::kBundlePolicyMaxBundle;
	rtcConfigure.rtcp_mux_policy = webrtc::PeerConnectionInterfaceDefs::kRtcpMuxPolicyRequire;
	rtcConfigure.continual_gathering_policy = webrtc::PeerConnectionInterfaceDefs::ContinualGatheringPolicy::GATHER_ONCE;	//GATHER_CONTINUALLY
	rtcConfigure.candidate_network_policy = webrtc::PeerConnectionInterfaceDefs::CandidateNetworkPolicy::kCandidateNetworkPolicyLowCost;
	rtcConfigure.tcp_candidate_policy = webrtc::PeerConnectionInterfaceDefs::TcpCandidatePolicy::kTcpCandidatePolicyDisabled;
	rtcConfigure.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	rtcConfigure.servers = _ice_servers;

	if (!_rtc_peer->CreateRTCConnection(rtcConfigure)) {
		_state = RTCClientInterface::RTC_CLIENT_STATE_ERROR;
		return INVALID_OPERATION;
	}

	_audio_sender = std::make_shared<RTCClientAudioSender>(7777, this);
	if(_audio_sender != nullptr) {
		_audio_sender->CreateTrack();
	}

	_state = RTCClientInterface::RTC_CLIENT_STATE_CREATE_CLIENT;
	return OK;
}

status_t RTCClientImpl::CreateAnswer() {
	_rtc_peer->CreateAnswer();
	return OK;
}

std::string& RTCClientImpl::GetId() {
	return _id;
}

status_t RTCClientImpl::SetRemoteDescription(const std::string &sdp) {
	std::unique_ptr<webrtc::SessionDescriptionInterface> remote_description;
	remote_description = webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp.c_str());

	if (remote_description == nullptr) {
		_state = RTCClientInterface::RTC_CLIENT_STATE_ERROR;
		return INVALID_OPERATION;
	}

	_rtc_peer->SetRemoteDescription(std::move(remote_description));
	_state = RTCClientInterface::RTC_CLIENT_STATE_SET_OFFER_DESCRIPTION;

	return OK;	
}

status_t RTCClientImpl::SetRemoteCandidate(int32_t mline_idx,
							   const std::string &mid, const std::string &candidate) {
	if (_state == RTCClientInterface::RTC_CLIENT_STATE_CREATE_CLIENT) {
		RTCCandidateInfo cinfo;
		cinfo._candidate = candidate.c_str();
		cinfo._sdp_index_line = mline_idx;
		cinfo._sdp_mid = mid.c_str();

		_remote_candidates.push_back(cinfo);
	}
	else {
		std::unique_ptr<webrtc::IceCandidateInterface> ice_candidate(
			webrtc::CreateIceCandidate(
			mid.c_str(), mline_idx, candidate.c_str(), nullptr));
		_rtc_peer->AddIceCandidate(ice_candidate.get());
	}

	return OK;
}

status_t RTCClientImpl::Close() {
	RTC_CLASS_FUNCTION_TRACER("RTCClient::Close", this);
	if (_rtc_peer) {
		if (_audio_receiver != nullptr) {
			_rtc_peer->SetAudioReceivedPacketSink(nullptr);
			_audio_receiver->close();
			_audio_receiver.reset();
		}
		if(_audio_sender != nullptr) {
			_audio_sender_scheduler->Unregister(_audio_sender.get());
			_audio_sender->RemoveTrack();
			_audio_sender.reset();
		}
		_rtc_peer->Close();
	}

	return OK;
}

RTCClientInterface::RTCClientState RTCClientImpl::GetState() {
	return _state;
}

RTCAudioIngressSinkSetInterface* RTCClientImpl::GetAudioIngressSetInterface() {
	if(_audio_receiver != nullptr) {
		return _audio_receiver.get();
	}
	return nullptr;
}

RTCAudioEgressSourceSetInterface* RTCClientImpl::GetAudioEgressSetInterface() {
	if(_audio_sender != nullptr) {
		return _audio_sender.get();
	}

	return nullptr;
}

void RTCClientImpl::OnIceConnectionChange(webrtc::PeerConnectionInterfaceDefs::IceConnectionState new_state) {
	ALOGD("OnIceConnectionChange new state %u", new_state);
	bool bchanged = false;
	if (new_state == webrtc::PeerConnectionInterfaceDefs::IceConnectionState::kIceConnectionChecking) {
		if (_ice_state != RTCClientListener::RTC_ICE_STATE_CONNECTING) {
			_ice_state = RTCClientListener::RTC_ICE_STATE_CONNECTING;
			bchanged = true;
		}
	} 
	else if (new_state == webrtc::PeerConnectionInterfaceDefs::IceConnectionState::kIceConnectionConnected ||
		new_state == webrtc::PeerConnectionInterfaceDefs::IceConnectionState::kIceConnectionCompleted) {
		if (_ice_state != RTCClientListener::RTC_ICE_STATE_CONNECTED) {
			_ice_state = RTCClientListener::RTC_ICE_STATE_CONNECTED;
			if(_audio_sender != nullptr) {
				_audio_sender_scheduler->Register(_audio_sender.get());
			}
			
			bchanged = true;
		}
	}
	else if (new_state == webrtc::PeerConnectionInterfaceDefs::IceConnectionState::kIceConnectionFailed) {
		if (_ice_state != RTCClientListener::RTC_ICE_STATE_FAILED) {
			_ice_state = RTCClientListener::RTC_ICE_STATE_FAILED;
			bchanged = true;
		}
	}
	else if (new_state == webrtc::PeerConnectionInterfaceDefs::IceConnectionState::kIceConnectionDisconnected) {
		if (_ice_state != RTCClientListener::RTC_ICE_STATE_DISCONNECTED) {
			_ice_state = RTCClientListener::RTC_ICE_STATE_DISCONNECTED;
			bchanged = true;
		}
	}
	else if (new_state == webrtc::PeerConnectionInterfaceDefs::IceConnectionState::kIceConnectionClosed) {
		if (_ice_state != RTCClientListener::RTC_ICE_STATE_CLOSED) {
			_ice_state = RTCClientListener::RTC_ICE_STATE_CLOSED;
			bchanged = true;
		}
	}

	if (bchanged) {
		_listener->OnConnectionState(_id, _ice_state);
	}
}

void RTCClientImpl::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {
	std::string std_candidate;
	candidate->ToString(&std_candidate);
	std::string mid = candidate->sdp_mid();
	_listener->OnLocalCandidate(_id, candidate->sdp_mline_index(), mid, std_candidate);
}

void RTCClientImpl::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnAddStream", this);
}

void RTCClientImpl::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnRemoveStream", this);
}

void RTCClientImpl::OnAddTrack(rtc::scoped_refptr<webrtc::RTCRtpReceiverInterface> receiver,
	const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) {
	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnAddTrack", this);
}

void RTCClientImpl::OnAddSimulcastTrack(std::vector<rtc::scoped_refptr<webrtc::RTCRtpReceiverInterface>>& receivers,
									const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) {
	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnAddSimulcastTrack", this);
}

void RTCClientImpl::OnTrack(rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> transceiver) {
//	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnTrack", this);

	if (transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_AUDIO) {
		ALOGI("[%x] RTCPeer::OnTrack audio", this);
//		_dump_transceiver(transceiver);
		_audio_receiver = std::make_shared<RTCClientAudioRtpReceiver>(transceiver, this);
		_rtc_peer->SetAudioReceivedPacketSink(_audio_receiver.get());

	} else if (transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO) {
		ALOGI("[%x] RTCPeer::OnTrack video", this);
	}
}

void RTCClientImpl::OnRemoveTrack(rtc::scoped_refptr<webrtc::RTCRtpReceiverInterface> receiver) {
	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnRemoveTrack", this);
}

void RTCClientImpl::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnDataChannel", this);
}


void RTCClientImpl::OnSdpCreateSuccess(webrtc::SessionDescriptionInterface *desc) {
	std::string sdp;
	desc->ToString(&sdp);

	std::unique_ptr<webrtc::SessionDescriptionInterface> local_description;
	local_description = webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp.c_str());

	if (local_description == nullptr) {
		std::string errdesc = "local description alloc fail";
		_listener->OnClientError(_id, errdesc);
		_state = RTCClientInterface::RTC_CLIENT_STATE_ERROR;
		return;
	}

	_rtc_peer->SetLocalDescription(std::move(local_description));
	_state = RTCClientInterface::RTC_CLIENT_STATE_SET_ANSWER_DESCRIPTION;
}

void RTCClientImpl::OnSdpCreateFailure(webrtc::RTCError error) {
	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnSdpCreateFailure", this);
	std::string errdesc = "local description create fail";
	_state = RTCClientInterface::RTC_CLIENT_STATE_ERROR;
	_listener->OnClientError(_id, errdesc);
}

void RTCClientImpl::OnSdpSetSuccess(bool is_local, webrtc::SessionDescriptionInterface* desc) {
//	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnSdpSetSuccess", this);
	if(is_local) {
		if(_audio_sender != nullptr) {
			_audio_sender->CheckSender();
		}

		if(_audio_receiver != nullptr) {
			_audio_receiver->getRtpParameters();
		}

		std::string sdp;
		desc->ToString(&sdp);
		_listener->OnLocalDescription(_id, sdp);
	} else {
		if (!_remote_candidates.empty()) {
			for (auto candidate : _remote_candidates) {
				std::unique_ptr<webrtc::IceCandidateInterface> ice_candidate(
					webrtc::CreateIceCandidate(
					candidate._sdp_mid, candidate._sdp_index_line, candidate._candidate, nullptr));
				_rtc_peer->AddIceCandidate(ice_candidate.get());
			}
			_remote_candidates.clear();
		}

		_state = RTCClientInterface::RTC_CLIENT_STATE_CREATE_ANSWER;
		_listener->OnSetRemoteDescription(_id);
	}
}

void RTCClientImpl::OnSdpSetFailure(webrtc::RTCError error, bool is_local) {
	RTC_CLASS_FUNCTION_TRACER("RTCClientImpl::OnSdpSetFailure", this);
	std::string errdesc = is_local ? "local" : "remote"; 
	errdesc += " description set fail";
	_state = RTCClientInterface::RTC_CLIENT_STATE_ERROR;		
	_listener->OnClientError(_id, errdesc);
}

void RTCClientImpl::_on_pcm_data(const int16_t* data, size_t samples, int sample_rate, int channels) {

}

void RTCClientImpl::_dump_transceiver(rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> transceiver) {
	ALOGV("[%x] _dump_transceiver +++++++++++++++++++++", this);
	cricket::MediaType media_type = transceiver->media_type();
	ALOGD("media_type  %s", media_type == cricket::MediaType::MEDIA_TYPE_AUDIO ? "audio" : "video");
	absl::optional<std::string> mid = transceiver->mid();
	if (mid != absl::nullopt) {
		ALOGD("mid %s", mid.value().c_str());
	}
	
	rtc::scoped_refptr<webrtc::RTCRtpReceiverInterface> receiver = transceiver->receiver();
	uint32_t ssrc = receiver->ssrc();
	ALOGD("ssrc %u", ssrc);
	webrtc::RtpParameters rtpParameters = receiver->GetParameters();

	ALOGD("******* RtpCodecParameters *******");
	for (auto it : rtpParameters.codecs) {
		ALOGD("mime_type %s", it.mime_type().c_str());
		ALOGD("name %s", it.name.c_str());
		ALOGD("payload_type %d", it.payload_type);
		if (it.clock_rate != absl::nullopt) {
			ALOGD("clock rate %d", it.clock_rate.value());
		}
		if (it.num_channels != absl::nullopt) {
			ALOGD("num_channels %d", it.num_channels.value());
		}
	}

	ALOGD("******* RtpExtension *******");
	for (auto it : rtpParameters.header_extensions) {
		std::string tmp = it.ToString();
		ALOGD("%s", tmp.c_str());
	}

	ALOGD("******* RtpEncodingParameters *******");
	for (auto it : rtpParameters.encodings) {
		if (it.ssrc != absl::nullopt) {
			ALOGD("ssrc %u", it.ssrc.value());
		}
	}

	ALOGV("[%x] _dump_transceiver ---------------------", this);
}
