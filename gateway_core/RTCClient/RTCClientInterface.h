//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_PEER_CLIENT_INTERFACE_H__
#define __RTC_PEER_CLIENT_INTERFACE_H__

#include <string>

#include "RTCAudioOuterSetInterface.h"

namespace webrtc {
class RTCPeer;
}

class RTCClientListener;	

class RTCClientInterface  {
public:
	typedef enum {
		RTC_CLIENT_STATE_UNINIT,
		RTC_CLIENT_STATE_INIT,
		RTC_CLIENT_STATE_CREATE_CLIENT,
		RTC_CLIENT_STATE_SET_OFFER_DESCRIPTION,
		RTC_CLIENT_STATE_CREATE_ANSWER,
		RTC_CLIENT_STATE_SET_ANSWER_DESCRIPTION,
		RTC_CLIENT_STATE_ERROR,
	} RTCClientState;

public:
	RTCClientInterface() = default;
	~RTCClientInterface() = default;

public:
	virtual webrtc::RTCPeer* PeerPtr() = 0;
	virtual status_t CreatePeerConnection() = 0;
	virtual status_t CreateAnswer() = 0;
	virtual std::string& GetId() = 0;
	virtual status_t SetRemoteDescription(const std::string &sdp) = 0;
	virtual status_t SetRemoteCandidate(int32_t mline_idx, const std::string &mid, const std::string &candidate) = 0;
	virtual status_t Close() = 0;
	virtual RTCClientState GetState() = 0;
	virtual RTCAudioIngressSinkSetInterface* GetAudioIngressSetInterface() = 0;
	virtual RTCAudioEgressSourceSetInterface* GetAudioEgressSetInterface() = 0;
};

#endif // __RTC_PEER_CLIENT_INTERFACE_H__