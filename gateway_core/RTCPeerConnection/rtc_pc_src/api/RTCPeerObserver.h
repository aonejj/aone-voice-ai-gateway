#ifndef __RTC_PEER_OBSERVER_H__
#define __RTC_PEER_OBSERVER_H__

#include "rtc_base/ref_count.h"
#include "api/rtc_error.h"
#include "api/jsep.h"
#include "api/data_channel_interface.h"

#include "../../src_update/api/_peer_connection_interface_defs.h"
#include "RTCRtpReceiverInterface.h"
#include "RTCRtpTransceiverInterface.h"



namespace webrtc {

class RTCPeerObserver {
public:
	virtual void OnAddStream(
		rtc::scoped_refptr<MediaStreamInterface> stream) {}
	virtual void OnRemoveStream(
		rtc::scoped_refptr<MediaStreamInterface> stream) {}

	virtual void OnIceConnectionChange(
		PeerConnectionInterfaceDefs::IceConnectionState new_state) {}

	virtual void OnIceCandidate(
		const IceCandidateInterface *candidate) = 0;

	// for planB sdp	
	virtual void OnAddTrack(
		rtc::scoped_refptr<RTCRtpReceiverInterface> receiver,
		const std::vector<rtc::scoped_refptr<MediaStreamInterface>>& streams) {}

	// for planB sdp
	virtual void OnAddSimulcastTrack(std::vector<rtc::scoped_refptr<RTCRtpReceiverInterface>>& receivers,
		const std::vector<rtc::scoped_refptr<MediaStreamInterface>>& streams) {}

	virtual void OnTrack(
		rtc::scoped_refptr<RTCRtpTransceiverInterface> transceiver) {}

	virtual void OnRemoveTrack(
		rtc::scoped_refptr<RTCRtpReceiverInterface> receiver) {}

	virtual void OnDataChannel(rtc::scoped_refptr<DataChannelInterface> data_channel) {}

	virtual void OnSdpCreateSuccess(SessionDescriptionInterface *desc) = 0;
	virtual void OnSdpCreateFailure(RTCError error) = 0;

	virtual void OnSdpSetSuccess(bool is_local, SessionDescriptionInterface* desc) = 0;
	virtual void OnSdpSetFailure(RTCError error, bool is_local) = 0;
};

}

#endif	// __RTC_PEER_OBSERVER_H__