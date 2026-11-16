//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_PEER_CLIENT_LISTENER_H__
#define __RTC_PEER_CLIENT_LISTENER_H__

#include <string>
#include <utils/Errors.h>

class RTCClientListener {
public:
	typedef enum {
		RTC_ICE_STATE_INIT,
		RTC_ICE_STATE_CONNECTING,
		RTC_ICE_STATE_CONNECTED,
		RTC_ICE_STATE_DISCONNECTED,
		RTC_ICE_STATE_CLOSED,
		RTC_ICE_STATE_FAILED,
	} RTCIceState;
public:
	RTCClientListener() = default;
	~RTCClientListener() = default;

public:
	virtual status_t OnLocalDescription(std::string &id, std::string &sdp) = 0;
	virtual status_t OnLocalCandidate(std::string &id, int32_t mline_idx, 
								 std::string &mid, std::string &candidate) = 0;
	virtual status_t OnConnectionState(std::string &id, RTCIceState ice_state) = 0;
	virtual status_t OnSetRemoteDescription(std::string &id) = 0;
	virtual status_t OnClientError(std::string &id, std::string& err) = 0;
};

#endif // __RTC_PEER_CLIENT_LISTENER_H__