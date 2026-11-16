//////////////////////////////////////////////////////////////////////////
//
// author : kimi
// desc   : peerconnection factory lite version for rtc media server.
//
//////////////////////////////////////////////////////////////////////////

#include <syscall.h>	// for __NR_gettid

#include "rtc_base/logging.h"
#include "rtc_base/checks.h"
#include "rtc_base/bind.h"

#include "api/rtc_error.h"
#include "api/data_channel_interface.h"
#include "rtc_base/ssl_adapter.h"

#include "../pc/RTCConnection.h"
#include "RTCReceivedPacketSinks.h"
#include "RTCRtpSenderInterface.h"
#include "RTCConnectionProxy.h"
#include "RTCPeer.h"

#include "../rtc_pc_src/RTCFunctionTracer.h"

namespace webrtc {

//////////////////////////////////////////////////////////////////////////
// RTCVideoRtpPacketLisenterProxyGroup
RTCPeer::RTCVideoRtpPacketLisenterProxyGroup::RTCVideoRtpPacketLisenterProxyGroup() {
	RTC_LOG(LS_VERBOSE) << "proxy_trace RTCVideoRtpPacketLisenterProxyGroup";
}

RTCPeer::RTCVideoRtpPacketLisenterProxyGroup::~RTCVideoRtpPacketLisenterProxyGroup() {
	RTC_LOG(LS_VERBOSE) << "proxy_trace ~RTCVideoRtpPacketLisenterProxyGroup";
	_proxies.clear();
}

void RTCPeer::RTCVideoRtpPacketLisenterProxyGroup::SetSink(RTCMediaReceivedPacketSink* sink) {
	for (auto it = _proxies.begin();
		it != _proxies.end(); ++it) {
		it->second->SetSink(sink);
	}
}

void RTCPeer::RTCVideoRtpPacketLisenterProxyGroup::PushVideoRtpPacketListenerProxy(uint32_t ssrc, std::unique_ptr<RTCVideoRtpPacketListenerProxy> proxy) {
//	_proxies.push_back(std::move(proxy));
	_proxies[ssrc] = std::move(proxy);
}

RTCVideoRtpPacketListenerProxy* RTCPeer::RTCVideoRtpPacketLisenterProxyGroup::GetProxy(uint32_t ssrc) {
	auto it = _proxies.find(ssrc);
	if (it == _proxies.end()) {
		return nullptr;
	}

	return it->second.get();
}


//////////////////////////////////////////////////////////////////////////
// RTCPeer
rtc::scoped_refptr<RTCPeer> RTCPeer::Create(RTCPeerObserver *observer) {
	rtc::scoped_refptr<RTCPeer> node(new rtc::RefCountedObject<RTCPeer>(observer));

	RTCError init_error = node->_initialize();
	if (!init_error.ok()) {
		RTC_LOG(LS_VERBOSE) << "RTCNode initialize failed";
		return nullptr;
	}

	return node;
}

bool RTCPeer::InitializeSSL() {
	return rtc::InitializeSSL();
}

bool RTCPeer::CleanupSSL() {
	return rtc::CleanupSSL();
} 

RTCPeer::RTCPeer(RTCPeerObserver *observer) 
	:_observer(observer)
{
//	RTC_CLASS_FUNCTION_TRACER("RTCPeer::RTCPeer", this);
}

RTCPeer::~RTCPeer() {
//	RTC_CLASS_FUNCTION_TRACER("RTCPeer::~RTCPeer", this);
	_close();

}

bool RTCPeer::CreateRTCConnection(
	PeerConnectionInterfaceDefs::RTCConfiguration &rtcConfigure) {

	if (signaling_thread() && !signaling_thread()->IsCurrent()) {
		return signaling_thread()->Invoke<bool>(
			RTC_FROM_HERE, [&] {
				return CreateRTCConnection(rtcConfigure);
		});
	}
	
	std::unique_ptr<RTCCall> call = worker_thread()->Invoke<std::unique_ptr<RTCCall>>(		// source
		RTC_FROM_HERE,
		rtc::Bind(&RTCPeer::_create_call_w, this));

	auto result = RTCConnection::Create(this, std::move(call),
												rtcConfigure, this, _thread_manager_ptr);

	if (result == nullptr)
		return false;

	_rtc_connection =
		RTCConnectionProxy::Create(signaling_thread(), std::move(result));

	_is_unified_plan = false;

	if (rtcConfigure.sdp_semantics == SdpSemantics::kUnifiedPlan) {
		_is_unified_plan = true;
	}

	return true;
}

void RTCPeer::CreateOffer() {
	if (_rtc_connection == nullptr) {
		RTC_LOG(LS_ERROR) << "["<<this<<"] RTCPeer::CreateOffer rtc connection NULL";
		return;
	}

	rtc::scoped_refptr<RTCCreateSdpObserver> 
		observer(new rtc::RefCountedObject<RTCCreateSdpObserver>(this));
	PeerConnectionInterfaceDefs::RTCOfferAnswerOptions options;
	options.offer_to_receive_audio = (_is_unified_plan ? PeerConnectionInterfaceDefs::RTCOfferAnswerOptions::kUndefined :
		PeerConnectionInterfaceDefs::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue);
	options.offer_to_receive_video = (_is_unified_plan ? PeerConnectionInterfaceDefs::RTCOfferAnswerOptions::kUndefined :
		PeerConnectionInterfaceDefs::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue);

	_rtc_connection->CreateOffer(observer, options);
}

void RTCPeer::CreateAnswer() {
	if (_rtc_connection == nullptr) {
		RTC_LOG(LS_ERROR) << "["<<this<<"] RTCPeer::CreateAnswer rtc connection NULL";
		return;
	}

	rtc::scoped_refptr<RTCCreateSdpObserver> 
		observer(new rtc::RefCountedObject<RTCCreateSdpObserver>(this));
	PeerConnectionInterfaceDefs::RTCOfferAnswerOptions options;
	options.offer_to_receive_audio = (_is_unified_plan ? PeerConnectionInterfaceDefs::RTCOfferAnswerOptions::kUndefined :
		PeerConnectionInterfaceDefs::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue);
	options.offer_to_receive_video = (_is_unified_plan ? PeerConnectionInterfaceDefs::RTCOfferAnswerOptions::kUndefined :
		PeerConnectionInterfaceDefs::RTCOfferAnswerOptions::kOfferToReceiveMediaTrue);

	_rtc_connection->CreateAnswer(observer, options);
}

void RTCPeer::SetLocalDescription(SessionDescriptionInterface *desc_ptr) {
	RTC_CLASS_FUNCTION_TRACER("RTCPeer::SetLocalDescription", this);
	if (_rtc_connection == nullptr) {
		RTC_LOG(LS_ERROR) << "[" << this <<"] RTCPeer::SetLocalDescription rtc connection NULL";
		return;
	}

	rtc::scoped_refptr<RTCSetSdpObserver> 
		observer(new rtc::RefCountedObject<RTCSetSdpObserver>(this, true, desc_ptr->Clone()));
	_rtc_connection->SetLocalDescription(observer, desc_ptr);
}

void RTCPeer::SetLocalDescription(
	std::unique_ptr<SessionDescriptionInterface> desc) {
	RTC_CLASS_FUNCTION_TRACER("RTCPeer::SetLocalDescription", this);
	if (_rtc_connection == nullptr) {
		RTC_LOG(LS_ERROR) << "["<<this<<"] RTCPeer::SetLocalDescription rtc connection NULL";
		return;
	}

	rtc::scoped_refptr<RTCSetSdpObserver>
		observer(new rtc::RefCountedObject<RTCSetSdpObserver>(this, true, desc->Clone()));

	_rtc_connection->SetLocalDescription(observer, std::move(desc));
}

void RTCPeer::SetRemoteDescription(SessionDescriptionInterface *desc_ptr) {
	if (_rtc_connection == nullptr) {
		RTC_LOG(LS_ERROR) << "["<<this<<"] RTCPeer::SetRemoteDescription rtc connection NULL";
		return;
	}

	rtc::scoped_refptr<RTCSetSdpObserver> 
		observer(new rtc::RefCountedObject<RTCSetSdpObserver>(this, false, nullptr));
	_rtc_connection->SetRemoteDescription(observer, desc_ptr);
}

void RTCPeer::SetRemoteDescription(
	std::unique_ptr<SessionDescriptionInterface> desc) {
	if (_rtc_connection == nullptr) {
		RTC_LOG(LS_ERROR) << "["<<this<<"] RTCPeer::SetRemoteDescription rtc connection NULL";
		return;
	}

	rtc::scoped_refptr<RTCSetSdpObserver>
		observer(new rtc::RefCountedObject<RTCSetSdpObserver>(this, false, nullptr));
	_rtc_connection->SetRemoteDescription(observer, std::move(desc));
}

void RTCPeer::AddIceCandidate(
	const IceCandidateInterface *candidate) {
	if (_rtc_connection == nullptr) {
		RTC_LOG(LS_ERROR) << "["<<this<<"] RTCPeer::AddIceCandidate rtc connection NULL";
		return;
	}

	_rtc_connection->AddIceCandidate(candidate);
}

void RTCPeer::AddIceCandidate(
	std::unique_ptr<IceCandidateInterface> candidate,
	std::function<void(RTCError)> callback) {
	// TODO....
}

rtc::scoped_refptr<rtc::RTCVideoRouteTrackSource> RTCPeer::CreateVideoRouteTrackSource() {
	rtc::scoped_refptr<rtc::RTCVideoRouteTrackSource> source(new rtc::RefCountedObject<rtc::RTCVideoRouteTrackSource>());

	return source;
}

rtc::scoped_refptr<RTCVideoRouteTrack> RTCPeer::CreateVideoRouteTrack(
	std::string &track_id, VideoTrackSourceInterface* source) {
	rtc::scoped_refptr<RTCVideoRouteTrack> track = RTCVideoRouteTrack::Create(track_id, source, worker_thread());

	return track;
}

rtc::scoped_refptr<rtc::RTCAudioRouteTrackSource> RTCPeer::CreateAudioRouteTrackSource() {
	rtc::scoped_refptr<rtc::RTCAudioRouteTrackSource> source(new rtc::RefCountedObject<rtc::RTCAudioRouteTrackSource>());

	return source;
}

rtc::scoped_refptr<RTCAudioRouteTrack> RTCPeer::CreateAudioRouteTrack(
	std::string& track_id, RTCAudioSourceInterface* source) {
	rtc::scoped_refptr<RTCAudioRouteTrack> track = RTCAudioRouteTrack::Create(track_id, source, worker_thread());

	return track;
}

rtc::scoped_refptr<RTCRtpTransceiverInterface> RTCPeer::AddTransceiver(
	rtc::scoped_refptr<MediaStreamTrackInterface> track) {
	RTCErrorOr<rtc::scoped_refptr<RTCRtpTransceiverInterface>> result =
		_rtc_connection->AddTransceiver(track);

	if (!result.ok()) {
		return nullptr;
	}

	return result.MoveValue();
}

rtc::scoped_refptr<RTCRtpTransceiverInterface> RTCPeer::AddTransceiver(
	rtc::scoped_refptr<MediaStreamTrackInterface> track,
	const RTCRtpTransceiverInit& init) {
	RTCErrorOr<rtc::scoped_refptr<RTCRtpTransceiverInterface>> result =
		_rtc_connection->AddTransceiver(track, init);

	if (!result.ok()) {
		return nullptr;
	}

	return result.MoveValue();
}

rtc::scoped_refptr<RTCRtpTransceiverInterface> RTCPeer::AddTransceiver(
	cricket::MediaType media_type) {
	RTCErrorOr<rtc::scoped_refptr<RTCRtpTransceiverInterface>> result =
		_rtc_connection->AddTransceiver(media_type);

	if (!result.ok()) {
		return nullptr;
	}

	return result.MoveValue();
}

rtc::scoped_refptr<RTCRtpTransceiverInterface> RTCPeer::AddTransceiver(
	cricket::MediaType media_type,
	const RTCRtpTransceiverInit& init) {
	RTCErrorOr<rtc::scoped_refptr<RTCRtpTransceiverInterface>> result =
		_rtc_connection->AddTransceiver(media_type, init);

	if (!result.ok()) {
		return nullptr;
	}

	return result.MoveValue();
}

rtc::scoped_refptr<RTCRtpSenderInterface> RTCPeer::AddTrack(
	rtc::scoped_refptr<MediaStreamTrackInterface> track,
	const std::vector<std::string>& stream_ids) {
	RTCErrorOr<rtc::scoped_refptr<RTCRtpSenderInterface>> result = 
		_rtc_connection->AddTrack(track, stream_ids);

	if (!result.ok()) {
		RTC_LOG(LS_ERROR) << "["<<this<<"] RTCNode::AddTrack Failed to add track: " << result.error().message();
		return nullptr;
	}

	return result.MoveValue();
}

bool RTCPeer::RemoveTrack(rtc::scoped_refptr<RTCRtpSenderInterface> sender) {
	return _rtc_connection->RemoveTrack(sender).ok();
}

std::vector<rtc::scoped_refptr<RTCRtpSenderInterface>> RTCPeer::GetSenders() const {
	return _rtc_connection->GetSenders();
}

std::vector<rtc::scoped_refptr<RTCRtpReceiverInterface>> RTCPeer::GetReceivers() const {
	return _rtc_connection->GetReceivers();
}

std::vector<rtc::scoped_refptr<RTCRtpTransceiverInterface>> RTCPeer::GetTransceivers() const {
	return _rtc_connection->GetTransceivers();
}

rtc::scoped_refptr<DataChannelInterface> RTCPeer::CreateDataChannel(const std::string& label) {
	return _rtc_connection->CreateDataChannel(label, nullptr);
}

void RTCPeer::SetAudioReceivedPacketSink(RTCMediaReceivedPacketSink* sink) {
	_audio_rtp_packet_listener_proxy->SetSink(sink);
}

void RTCPeer::SetVideoReceivedPacketSink(RTCMediaReceivedPacketSink* sink) {
	auto it = _proxy_groups.find(sink);
	if (it != _proxy_groups.end()) {
		it->second->SetSink(sink);
	}
}

void RTCPeer::ResetVideoReceivedPacketSink(RTCMediaReceivedPacketSink* sink) {
	auto it = _proxy_groups.find(sink);
	if (it != _proxy_groups.end()) {
		it->second->SetSink(nullptr);
	}
}

bool RTCPeer::CreateVideoReceivedPacketListenerProxyGroup(RTCMediaReceivedPacketSink* sink,
	std::vector<uint32_t>& ssrcs) {

	_proxy_groups[sink] = std::unique_ptr<RTCVideoRtpPacketLisenterProxyGroup>(new RTCVideoRtpPacketLisenterProxyGroup());
	for (auto it : ssrcs) {
		_proxy_groups[sink]->PushVideoRtpPacketListenerProxy(it, _get_video_rtp_packet_listener_proxy(it));
	}

	return true;
}

void RTCPeer::GenerateKeyFrame() {
	if (_rtc_connection != nullptr) {
		_rtc_connection->GenerateKeyFrame();
	}
}

void RTCPeer::GenerateKeyFramePli(uint32_t ssrc) {
	if (_rtc_connection != nullptr) {
		_rtc_connection->GenerateKeyFramePli(ssrc);
	}
}

RTCCall::Stats RTCPeer::GetCallStats() {
	if (_rtc_connection != nullptr) {
		return _rtc_connection->GetCallStats();
	}

	return RTCCall::Stats();
}

void RTCPeer::Close() {
	RTC_CLASS_FUNCTION_TRACER("RTCPeer::Close", this);
	_close();
}

rtc::Thread* RTCPeer::signaling_thread() {
	if(_thread_manager_ptr == nullptr) {
		return nullptr;
	}
	return _thread_manager_ptr->signaling_thread();
}

rtc::Thread* RTCPeer::worker_thread() {
	if(_thread_manager_ptr == nullptr) {
		return nullptr;
	}
	return _thread_manager_ptr->worker_thread();
}

rtc::Thread* RTCPeer::network_thread() {
	if(_thread_manager_ptr == nullptr) {
		return nullptr;
	}
	return _thread_manager_ptr->network_thread();
}

RTCError RTCPeer::_initialize() {
	_thread_manager_ptr = rtc::RTCThreadManagerPool::Instance().GetThreadManager();

	_call_factory = CreateCallFactory();

	_audio_rtp_packet_listener_proxy = std::unique_ptr<RTCAudioRtpPacketListenerProxy>(new RTCAudioRtpPacketListenerProxy());

	return RTCError::OK();
}

std::unique_ptr<RTCCall> RTCPeer::_create_call_w() {
	RTC_DCHECK_RUN_ON(worker_thread());
	return std::unique_ptr<RTCCall>(_call_factory->CreateRTCCall(_audio_rtp_packet_listener_proxy.get(),
																		 this
																		  , _thread_manager_ptr
																		 ));
}


std::unique_ptr<RTCVideoRtpPacketListenerProxy> 
RTCPeer::_get_video_rtp_packet_listener_proxy(uint32_t ssrc) {
	std::unique_ptr<RTCVideoRtpPacketListenerProxy> proxy;
	auto it = _video_rtp_packet_listener_proxies.find(ssrc);
	if (it == _video_rtp_packet_listener_proxies.end()) {
		return nullptr;
	}

	proxy = std::move(it->second);
	_video_rtp_packet_listener_proxies.erase(it);

	return std::move(proxy);
}

void RTCPeer::_close() {
	RTC_LOG(LS_INFO) << "[" << this << "] thread_crash_trace RTCPeer::_close+++ "; 

	if (_rtc_connection != nullptr) {
		_rtc_connection->Close();
		_rtc_connection = nullptr;
	}

	if (!_video_rtp_packet_listener_proxies.empty()) {
		_video_rtp_packet_listener_proxies.clear();
	}

	if (_network_thread != nullptr)
		_network_thread = nullptr;

	if (_worker_thread != nullptr)
		_worker_thread = nullptr;

	if (_signaling_thread != nullptr)
		_signaling_thread = nullptr;

	RTC_LOG(LS_INFO) << "[" << this << "] thread_crash_trace RTCPeer::_close--- ";
}

//////////////////////////////////////////////////////////////////////////
// RTCConnectionObserver
void RTCPeer::OnSignalingChange(PeerConnectionInterfaceDefs::SignalingState new_state) {
	RTC_LOG(LS_INFO) << "[" << this << "] RTCPeer::OnSignalingChange new_state " << new_state;
}

void RTCPeer::OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream) {
	if (_observer) {
		_observer->OnAddStream(stream);
	}
}

void RTCPeer::OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface> stream) {
	if (_observer) {
		_observer->OnRemoveStream(stream);
	}
}

void RTCPeer::OnDataChannel(rtc::scoped_refptr<DataChannelInterface> data_channel) {
	RTC_LOG(LS_VERBOSE) << "RTCPeer::OnDataChannel lable " << data_channel->label().c_str();
	if (_observer) {
		_observer->OnDataChannel(data_channel);
	}
}

void RTCPeer::OnRenegotiationNeeded() {

}

void RTCPeer::OnNegotiationNeededEvent(uint32_t event_id) {

}

void RTCPeer::OnIceConnectionChange(
	PeerConnectionInterfaceDefs::IceConnectionState new_state) {
	if (_observer) {
		_observer->OnIceConnectionChange(new_state);
	}
}

void RTCPeer::OnStandardizedIceConnectionChange(
	PeerConnectionInterfaceDefs::IceConnectionState new_state) {

}

void RTCPeer::OnConnectionChange(
	PeerConnectionInterfaceDefs::PeerConnectionState new_state) {

}

void RTCPeer::OnIceGatheringChange(
	PeerConnectionInterfaceDefs::IceGatheringState new_state) {

}

void RTCPeer::OnIceCandidate(const IceCandidateInterface* candidate) {
	if (candidate) {
		std::string out;
		candidate->ToString(&out);
		RTC_LOG(LS_VERBOSE) << "OnIceCandidate : " << out;
		if (_observer) {
			_observer->OnIceCandidate(candidate);
		}
	}
	else {
		RTC_LOG(LS_VERBOSE) << "OnIceCandidate : nullptr";
	}
}

void RTCPeer::OnIceCandidateError(const std::string& host_candidate,
								  const std::string& url,
								  int error_code,
								  const std::string& error_text) {

}

void RTCPeer::OnIceCandidateError(const std::string& address,
								  int port,
								  const std::string& url,
								  int error_code,
								  const std::string& error_text) {

}

void RTCPeer::OnIceCandidatesRemoved(
	const std::vector<cricket::Candidate>& candidates) {

}

void RTCPeer::OnIceConnectionReceivingChange(bool receiving) {

}

void RTCPeer::OnIceSelectedCandidatePairChanged(
	const cricket::CandidatePairChangeEvent& event) {

}

void RTCPeer::OnAddTrack(
	rtc::scoped_refptr<RTCRtpReceiverInterface> receiver,
	const std::vector<rtc::scoped_refptr<MediaStreamInterface>>& streams) {

	if (_observer) {
		_observer->OnAddTrack(receiver, streams);
	}
}

void RTCPeer::OnAddSimulcastTrack(std::vector<rtc::scoped_refptr<RTCRtpReceiverInterface>>& receivers,
	const std::vector<rtc::scoped_refptr<MediaStreamInterface>>& streams) {

	if (_observer) {
		_observer->OnAddSimulcastTrack(receivers, streams);
	}
}

void RTCPeer::OnTrack(rtc::scoped_refptr<RTCRtpTransceiverInterface> transceiver) {
	if (_observer) {
		_observer->OnTrack(transceiver);
	}
}

//////////////////////////////////////////////////////////////////////////
// RTCSdpObserver interface
void RTCPeer::OnCreateSuccess(SessionDescriptionInterface* desc) {
	RTC_LOG(LS_VERBOSE) << "["<<this<<"] RTCPeer::OnCreateSuccess";

	if (_observer) {
		_observer->OnSdpCreateSuccess(desc);
	}
}

void RTCPeer::OnCreateFailure(RTCError error) {
	RTC_LOG(LS_VERBOSE) << "["<<this<<"] OnCreateFailure";
	if (_observer) {
		_observer->OnSdpCreateFailure(error);
	}
}

void RTCPeer::OnSetSuccess(bool is_local, SessionDescriptionInterface* desc) {
	if (_observer) {
		_observer->OnSdpSetSuccess(is_local, desc);
	}
}
void RTCPeer::OnSetFailure(RTCError error, bool is_local) {
	RTC_LOG(LS_VERBOSE) << "["<<this<<"] RTCPeer::OnSetFailure";
	if (_observer) {
		_observer->OnSdpSetFailure(error, is_local);
	}
}

RTCVideoRtpPacketListenSinkInterface* RTCPeer::GetVideoRtpPacketProxyInterface(uint32_t ssrc) {
	for (auto it = _proxy_groups.begin();
		it != _proxy_groups.end(); ++it) {
		RTCVideoRtpPacketListenerProxy* proxy = it->second->GetProxy(ssrc);
		if (proxy != nullptr) {
			RTC_LOG(LS_VERBOSE) << "proxy_trace RTCPeer::GetVideoRtpPacketProxyInterface--- find proxy group  ssrc " << ssrc;
			return proxy;
		}
	}

	_video_rtp_packet_listener_proxies[ssrc] = 
		std::unique_ptr<RTCVideoRtpPacketListenerProxy>(new RTCVideoRtpPacketListenerProxy());
	
	return _video_rtp_packet_listener_proxies[ssrc].get();
}

}