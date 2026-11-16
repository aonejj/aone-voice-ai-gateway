//////////////////////////////////////////////////////////////////////////
//
// author : kimi
// desc   : peerconnection factory lite version for rtc media server.
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_PEER_H__
#define __RTC_PEER_H__

#include <stdint.h>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <map>

#include "../_deprecate_defines.h"

#include "api/rtc_error.h"
#include "api/scoped_refptr.h"
#include "rtc_base/thread.h"
#include "rtc_base/message_handler.h"
#include "rtc_base/thread_message.h"
#include "rtc_base/ref_count.h"

#include "../../src_update/api/_media_stream_interface.h"
#include "../../src_update/api/_peer_connection_interface_defs.h"

#include "_rtc_media_stream_interface.h"
#include "RTCConnectionInterface.h"
#include "ThreadsInfoInterface.h"
#include "RTCRtpReceiverInterface.h"
#include "./call/RTCCallFactoryInterface.h"
#include "./audio/RTCAudioRtpPacketListenerProxy.h"
#include "./video/RTCVideoRtpPacketListenerProxy.h"
#include "RTCPeerObserver.h"
#include "../call/RTCCall.h"
#include "../media/base/RTCVideoRouteTrackSource.h"
#include "../media/base/RTCAudioRouteTrackSource.h"
#include "../pc/RTCVideoRouteTrack.h"
#include "../pc/RTCAudioRouteTrack.h"
#include "../pc/RTCCreateSdpObserver.h"


#include "../RTCThreadManagerInterface.h"
#include "../RTCInvocableMessageHandler.h"



#include "../rtc_thread_provider/RTCThreadManagerPool.h"
#include "../rtc_thread_provider/RTCThreadManager.h"

namespace webrtc {

class RTCRtpSenderInterface;
class RTCMediaReceivedPacketSink;
class DataChannelInterface;

class RTCPeer : public rtc::RefCountInterface,
				public ThreadsInfoInterface,
				public RTCConnectionObserver,
				public RTCSdpObserver,
				public RTCVideeRtpPacketListenerProxyGetInterface {
public:
	static rtc::scoped_refptr<RTCPeer> Create(RTCPeerObserver *observer);
	static bool InitializeSSL();
	static bool CleanupSSL(); 

	bool CreateRTCConnection(PeerConnectionInterfaceDefs::RTCConfiguration &rtcConfigure);

	void CreateOffer();
	void CreateAnswer();
	void SetLocalDescription(SessionDescriptionInterface *desc_ptr);
	void SetLocalDescription(
		std::unique_ptr<SessionDescriptionInterface> desc);
	void SetRemoteDescription(SessionDescriptionInterface *desc_ptr);
	void SetRemoteDescription(
		std::unique_ptr<SessionDescriptionInterface> desc);
	void AddIceCandidate(
		const IceCandidateInterface *candidate);
	void AddIceCandidate(
		std::unique_ptr<IceCandidateInterface> candidate,
		std::function<void(RTCError)> callback);

	rtc::scoped_refptr<rtc::RTCVideoRouteTrackSource> CreateVideoRouteTrackSource();
	rtc::scoped_refptr<RTCVideoRouteTrack> CreateVideoRouteTrack(
						std::string &track_id, VideoTrackSourceInterface* source);

	rtc::scoped_refptr<rtc::RTCAudioRouteTrackSource> CreateAudioRouteTrackSource();
	rtc::scoped_refptr<RTCAudioRouteTrack> CreateAudioRouteTrack(
						std::string& track_id, RTCAudioSourceInterface* source);

	rtc::scoped_refptr<RTCRtpTransceiverInterface> AddTransceiver(
		rtc::scoped_refptr<MediaStreamTrackInterface> track);
	rtc::scoped_refptr<RTCRtpTransceiverInterface> AddTransceiver(
		rtc::scoped_refptr<MediaStreamTrackInterface> track,
		const RTCRtpTransceiverInit& init);
	rtc::scoped_refptr<RTCRtpTransceiverInterface> AddTransceiver(
		cricket::MediaType media_type);
	rtc::scoped_refptr<RTCRtpTransceiverInterface> AddTransceiver(
		cricket::MediaType media_type, 
		const RTCRtpTransceiverInit& init);

	rtc::scoped_refptr<RTCRtpSenderInterface> AddTrack(
		rtc::scoped_refptr<MediaStreamTrackInterface> track,
		const std::vector<std::string>& stream_ids);
	bool RemoveTrack(rtc::scoped_refptr<RTCRtpSenderInterface> sender);

	std::vector<rtc::scoped_refptr<RTCRtpSenderInterface>> GetSenders() const;
	std::vector<rtc::scoped_refptr<RTCRtpReceiverInterface>> GetReceivers() const;
	std::vector<rtc::scoped_refptr<RTCRtpTransceiverInterface>> GetTransceivers() const;

	rtc::scoped_refptr<DataChannelInterface> CreateDataChannel(const std::string& label);

	void SetAudioReceivedPacketSink(RTCMediaReceivedPacketSink* sink);
	void SetVideoReceivedPacketSink(RTCMediaReceivedPacketSink* sink);
	void ResetVideoReceivedPacketSink(RTCMediaReceivedPacketSink* sink);
	bool CreateVideoReceivedPacketListenerProxyGroup(RTCMediaReceivedPacketSink* sink,
													 std::vector<uint32_t>& ssrcs);

	void GenerateKeyFrame();
	void GenerateKeyFramePli(uint32_t ssrc);

	RTCCall::Stats GetCallStats();

	void Close();

public:
	rtc::Thread* signaling_thread() override;
	rtc::Thread* worker_thread() override;
	rtc::Thread* network_thread() override;

	// RTCConnectionObserver
	void OnSignalingChange(PeerConnectionInterfaceDefs::SignalingState new_state) override;
	void OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream) override;
	void OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface> stream) override;
	void OnDataChannel(rtc::scoped_refptr<DataChannelInterface> data_channel) override;
	void OnRenegotiationNeeded() override;
	void OnNegotiationNeededEvent(uint32_t event_id) override;
	void OnIceConnectionChange(
		PeerConnectionInterfaceDefs::IceConnectionState new_state) override;
	void OnStandardizedIceConnectionChange(
		PeerConnectionInterfaceDefs::IceConnectionState new_state) override;
	void OnConnectionChange(PeerConnectionInterfaceDefs::PeerConnectionState new_state) override;
	void OnIceGatheringChange(PeerConnectionInterfaceDefs::IceGatheringState new_state) override;
	void OnIceCandidate(const IceCandidateInterface* candidate) override;
	void OnIceCandidateError(const std::string& host_candidate,
		const std::string& url,
		int error_code,
		const std::string& error_text) override;
	void OnIceCandidateError(const std::string& address,
		int port,
		const std::string& url,
		int error_code,
		const std::string& error_text) override;
	void OnIceCandidatesRemoved(
		const std::vector<cricket::Candidate>& candidates) override;
	void OnIceConnectionReceivingChange(bool receiving) override;
	void OnIceSelectedCandidatePairChanged(
		const cricket::CandidatePairChangeEvent& event) override;

	void OnAddTrack(
		rtc::scoped_refptr<RTCRtpReceiverInterface> receiver,
		const std::vector<rtc::scoped_refptr<MediaStreamInterface>>& streams) override;

	void OnAddSimulcastTrack(std::vector<rtc::scoped_refptr<RTCRtpReceiverInterface>>& receivers,
			const std::vector<rtc::scoped_refptr<MediaStreamInterface>>& streams) override;

	void OnTrack(rtc::scoped_refptr<RTCRtpTransceiverInterface> transceiver) override;

	// RTCSdpObserver 
	void OnCreateSuccess(SessionDescriptionInterface* desc) override;
	void OnCreateFailure(RTCError error) override;
	void OnSetSuccess(bool is_local, SessionDescriptionInterface* desc) override;
	void OnSetFailure(RTCError error, bool is_local) override;

	// RTCVideeRtpPacketListenerProxyGetInterface
	RTCVideoRtpPacketListenSinkInterface* GetVideoRtpPacketProxyInterface(uint32_t ssrc) override;

protected:
	RTCPeer(RTCPeerObserver *observer);
	virtual ~RTCPeer() override;

private:
	class RTCVideoRtpPacketLisenterProxyGroup {
	public:
		RTCVideoRtpPacketLisenterProxyGroup();
		~RTCVideoRtpPacketLisenterProxyGroup();

	public:
		void SetSink(RTCMediaReceivedPacketSink* sink);
		void PushVideoRtpPacketListenerProxy(uint32_t ssrc, std::unique_ptr<RTCVideoRtpPacketListenerProxy> proxy);
		RTCVideoRtpPacketListenerProxy* GetProxy(uint32_t ssrc);

	private:
		std::map<uint32_t, std::unique_ptr<RTCVideoRtpPacketListenerProxy>> _proxies;
	};

private:
	RTCError _initialize();
	std::unique_ptr<RTCCall> _create_call_w();
	std::unique_ptr<RTCVideoRtpPacketListenerProxy> _get_video_rtp_packet_listener_proxy(uint32_t ssrc);
	void _close();

private:
	// threads
	std::unique_ptr<rtc::Thread> _network_thread;
	std::unique_ptr<rtc::Thread> _worker_thread;
	std::unique_ptr<rtc::Thread> _signaling_thread;
	std::unique_ptr<RTCCallFactoryInterface> _call_factory;

	std::unique_ptr<RTCAudioRtpPacketListenerProxy> _audio_rtp_packet_listener_proxy;
	
	std::map<uint32_t, std::unique_ptr<RTCVideoRtpPacketListenerProxy>> _video_rtp_packet_listener_proxies;

	std::map<RTCMediaReceivedPacketSink*, std::unique_ptr<RTCVideoRtpPacketLisenterProxyGroup>> _proxy_groups;

	RTCPeerObserver	*_observer;

	rtc::scoped_refptr<RTCConnectionInterface> _rtc_connection;

	bool _is_unified_plan;

	rtc::RTCThreadManager* _thread_manager_ptr;
};

}

#endif // __RTC_PEER_H__